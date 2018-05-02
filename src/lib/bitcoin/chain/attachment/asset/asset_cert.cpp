/**
 * Copyright (c) 2016-2018 metaverse core developers (see MVS-AUTHORS)
 *
 * This file is part of metaverse.
 *
 * metaverse is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <metaverse/bitcoin/chain/attachment/asset/asset_cert.hpp>
#include <sstream>
#include <metaverse/bitcoin/utility/container_sink.hpp>
#include <metaverse/bitcoin/utility/container_source.hpp>
#include <metaverse/bitcoin/utility/istream_reader.hpp>
#include <metaverse/bitcoin/utility/ostream_writer.hpp>
#include <metaverse/blockchain/block_chain_impl.hpp>
#include <metaverse/blockchain/validate_transaction.hpp>

namespace libbitcoin {
namespace chain {

constexpr bool use_did_address = false;
#define ASSET_SYMBOL_DELIMITER "."

asset_cert::asset_cert()
{
    reset();
}

asset_cert::asset_cert(std::string symbol, std::string owner, asset_cert_type certs)
    : symbol_(symbol)
    , owner_(owner)
    , certs_(certs)
{
}

void asset_cert::reset()
{
    symbol_ = "";
    owner_ = "";
    certs_ = asset_cert_ns::none;
}

bool asset_cert::is_valid() const
{
    return !(symbol_.empty()
            || owner_.empty()
            || (certs_ == asset_cert_ns::none)
            || ((symbol_.size()+1) > ASSET_CERT_SYMBOL_FIX_SIZE)
            || ((owner_.size()+1) > ASSET_CERT_OWNER_FIX_SIZE)
            );
}

bool asset_cert::operator< (const asset_cert& other) const
{
    return (symbol_ < other.symbol_)
        || ((symbol_ == other.symbol_) && (certs_ < other.certs_));
}

std::string asset_cert::get_domain(const std::string& symbol)
{
    std::string domain("");
    auto&& tokens = bc::split(symbol, ASSET_SYMBOL_DELIMITER, true);
    if (tokens.size() > 0) {
        domain = tokens[0];
    }
    return domain;
}

bool asset_cert::is_valid_domain(const std::string& domain)
{
    return !domain.empty();
}

asset_cert asset_cert::factory_from_data(const data_chunk& data)
{
    asset_cert instance;
    instance.from_data(data);
    return instance;
}

asset_cert asset_cert::factory_from_data(std::istream& stream)
{
    asset_cert instance;
    instance.from_data(stream);
    return instance;
}

asset_cert asset_cert::factory_from_data(reader& source)
{
    asset_cert instance;
    instance.from_data(source);
    return instance;
}

bool asset_cert::from_data(const data_chunk& data)
{
    data_source istream(data);
    return from_data(istream);
}

bool asset_cert::from_data(std::istream& stream)
{
    istream_reader source(stream);
    return from_data(source);
}

bool asset_cert::from_data(reader& source)
{
    reset();
    symbol_ = source.read_string();
    owner_ = source.read_string();
    certs_ = source.read_8_bytes_little_endian();

    auto result = static_cast<bool>(source);
    if (!result)
        reset();

    return result;
}

data_chunk asset_cert::to_data() const
{
    data_chunk data;
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    return data;
}

void asset_cert::to_data(std::ostream& stream) const
{
    ostream_writer sink(stream);
    to_data(sink);
}

void asset_cert::to_data(writer& sink) const
{
    sink.write_string(symbol_);
    sink.write_string(owner_);
    sink.write_8_bytes_little_endian(certs_);
}

uint64_t asset_cert::serialized_size() const
{
    size_t len = (symbol_.size()+1) + (owner_.size()+1) + ASSET_CERT_CERTS_FIX_SIZE;
    return std::min(len, ASSET_CERT_FIX_SIZE);
}

std::string asset_cert::to_string() const
{
    std::ostringstream ss;
    ss << "\t symbol = " << symbol_ << "\n";
    ss << "\t owner = " << owner_ << "\n";
    ss << "\t certs = " << std::to_string(get_certs()) << "\n";
    return ss.str();
}

const std::string& asset_cert::get_symbol() const
{
    return symbol_;
}

void asset_cert::set_symbol(const std::string& symbol)
{
    size_t len = std::min((symbol.size()+1), ASSET_CERT_SYMBOL_FIX_SIZE);
    symbol_ = symbol.substr(0, len);
}

const std::string& asset_cert::get_owner() const
{
    return owner_;
}

void asset_cert::set_owner(const std::string& owner)
{
    size_t len = std::min((owner.size()+1), ASSET_CERT_OWNER_FIX_SIZE);
    owner_ = owner.substr(0, len);
}

asset_cert_type asset_cert::get_certs() const
{
    return certs_;
}

void asset_cert::set_certs(asset_cert_type certs)
{
    certs_ = certs;
}

bool asset_cert::test_certs(asset_cert_type bits) const
{
    return test_certs(certs_, bits);
}

bool asset_cert::test_certs(asset_cert_type certs, asset_cert_type bits)
{
    return (certs & bits) == bits;
}

std::string asset_cert::get_address(bc::blockchain::block_chain_impl& chain) const
{
    if (!use_did_address || !bc::blockchain::validate_transaction::is_did_validate(chain)) {
        return owner_;
    }
    auto did_symbol = owner_;
    auto sp_did_detail = chain.get_issued_did(did_symbol);
    if (sp_did_detail) {
        return sp_did_detail->get_address();
    }
    return owner_;
}

std::string asset_cert::get_owner_from_address(bc::blockchain::block_chain_impl& chain) const
{
    return get_owner_from_address(owner_, chain);
}

std::string asset_cert::get_owner_from_address(const std::string& address,
        bc::blockchain::block_chain_impl& chain)
{
    // don't convert to did-symbol if did is not enabled.
    if (!use_did_address || !bc::blockchain::validate_transaction::is_did_validate(chain)) {
        return address;
    }
    return chain.get_did_from_address(address);
}

bool asset_cert::check_cert_owner(bc::blockchain::block_chain_impl& chain) const
{
    if (owner_.empty()) {
        return false;
    }
    // don't check did existence if did is not enabled.
    if (!use_did_address || !bc::blockchain::validate_transaction::is_did_validate(chain)) {
        return true;
    }
    return chain.is_did_exist(owner_);
}

} // namspace chain
} // namspace libbitcoin
